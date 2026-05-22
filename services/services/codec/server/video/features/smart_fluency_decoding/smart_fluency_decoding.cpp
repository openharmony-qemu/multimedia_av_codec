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
#include "avcodec_log_ex.h"
#include "buffer/avbuffer.h"
#include "buffer_extra_data_impl.h"
#include "double_utils.h"
#include "strategies/retention_strategy_factory.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "smart_fluency_decoding_types.h"
#include "surface_buffer.h"
#include "v2_0/buffer_handle_meta_key_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SmartFluencyDecoding"};
constexpr double DEFAULT_PLAYBACK_SPEED = 1.0;

#ifdef SFD_DEBUG
inline int64_t GetCurrentSysTimeUs()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}
#endif
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
namespace SFD {

int32_t SmartFluencyDecoding::Initialize(const SFDConfig& config)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(config.width > 0 && config.height > 0, AVCS_ERR_INVALID_VAL,
        "Invalid video dimensions: %{public}dx%{public}d", config.width, config.height);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(config.codecType != SFDCodecType::INVALID,
        AVCS_ERR_INVALID_VAL, "Unsupported codec type");
    config_ = config;
    int32_t ret = InitializeInternal();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == 0, AVCS_ERR_UNKNOWN, "Failed to initialize internal components");
    isInitialized_ = true;
    AVCODEC_LOGI_WITH_TAG("sfd initialized: %{public}dx%{public}d, codecType=%{public}d, mode=%{public}d",
        config.width, config.height, static_cast<int32_t>(config.codecType), static_cast<int32_t>(config.initMode));
    return AVCS_ERR_OK;
}

int32_t SmartFluencyDecoding::InitializeInternal()
{
    strategy_ = RetentionStrategyFactory::CreateStrategy(config_.initMode, config_.retentionRatio);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(strategy_ != nullptr, AVCS_ERR_UNKNOWN, "Failed to create strategy");
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
    AVCODEC_LOGI_WITH_TAG("Internal init completed: %{public}dx%{public}d, codecType=%{public}d",
        config_.width, config_.height, static_cast<int>(config_.codecType));
    return AVCS_ERR_OK;
}

void SmartFluencyDecoding::UpdatePlaybackSpeed(double speed)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    if (strategy_) {
        strategy_->UpdatePlaybackSpeed(speed);
#ifdef SFD_DEBUG
        if (dumper_) {
            dumper_->UpdateSpeed(speed);
        }
#endif
        AVCODEC_LOGI_WITH_TAG("SFD Playback speed dynamically updated to: %{public}.3f", speed);
    }
}

void SmartFluencyDecoding::OnDecInfoUpdate(double sourceFps, double speed)
{
    UpdatePlaybackSpeed(speed);
    std::lock_guard<std::mutex> lock(configMutex_);
    if (strategy_) {
        strategy_->UpdateSourceFramerate(sourceFps);
    }
}

void SmartFluencyDecoding::ParseRetentionConfig(
    const Media::Format& format, RetentionStrategyType& mode, std::optional<double>& ratio)
{
    bool hasNewRatio = false;
    double parsedRatio = 0.0;
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, parsedRatio)) {
        if (IsRatioValid(parsedRatio)) {
            ratio = parsedRatio;
            hasNewRatio = true;
            AVCODEC_LOGD("ParseRetentionConfig: Extracted valid ratio %{public}.3f", parsedRatio);
        }
    }
    int32_t extMode = -1;
    if (format.GetIntValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, extMode)) {
        switch (extMode) {
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::FULL):
                mode = RetentionStrategyType::FULL;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::ADAPTIVE):
                mode = RetentionStrategyType::ADAPTIVE;
                break;
            case static_cast<int32_t>(Media::Plugins::FrameRetentionMode::UNIFORM):
                mode = ratio.has_value() ? RetentionStrategyType::FIXED_RATIO : RetentionStrategyType::AUTO_RATIO;
                break;
            default:
                mode = RetentionStrategyType::INVALID;
                break;
        }
        AVCODEC_LOGD("ParseRetentionConfig: Extracted mode %{public}d -> internal %{public}d",
            extMode, static_cast<int>(mode));
    } else if (hasNewRatio) {
        if (mode == RetentionStrategyType::INVALID || mode == RetentionStrategyType::AUTO_RATIO) {
            mode = RetentionStrategyType::FIXED_RATIO;
        }
    }
    if (ratio.has_value() && (mode == RetentionStrategyType::ADAPTIVE || mode == RetentionStrategyType::FULL)) {
        AVCODEC_LOGW("ParseRetentionConfig: Adaptive or Full mode with retentionRate will be ignored");
    }
}

void SmartFluencyDecoding::ApplyNewStrategy(RetentionStrategyType newMode, const std::optional<double>& newRatio)
{
    auto newStrategy = RetentionStrategyFactory::CreateStrategy(newMode, newRatio);
    if (newStrategy == nullptr) {
        AVCODEC_LOGE_WITH_TAG("Failed to create new retention strategy (mode: %{public}d). Aborting update.",
            static_cast<int32_t>(newMode));
        return;
    }
    if (newStrategy->RequiresNaluAnalysis()) {
        EnsureNaluAnalyzer();
    }
    if (newStrategy->RequiresMvAnalysis()) {
        EnsureMvAnalyzer();
    }
    std::lock_guard<std::mutex> lock(configMutex_);
    strategy_ = std::move(newStrategy);
    AVCODEC_LOGI_WITH_TAG("SFD mode updated from %{public}d to %{public}d, ratio: %{public}.3f",
        static_cast<int32_t>(config_.initMode), static_cast<int32_t>(newMode),
        (newRatio.has_value() ? newRatio.value() : -1.0));
    config_.initMode = newMode;
    config_.retentionRatio = newRatio;
#ifdef SFD_DEBUG
    if (dumper_) {
        dumper_->UpdateMode(newMode, newRatio.has_value() ? newRatio.value() : DEFAULT_PLAYBACK_SPEED);
    }
#endif
}

int32_t SmartFluencyDecoding::UpdateDynamicConfig(const Media::Format& format)
{
    RetentionStrategyType newMode = config_.initMode;
    std::optional<double> newRatio = config_.retentionRatio;
    ParseRetentionConfig(format, newMode, newRatio);
    double speed = DEFAULT_PLAYBACK_SPEED;
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_SPEED, speed)) {
        AVCODEC_LOGD_WITH_TAG("UpdateDynamicConfig: Speed update detected: %{public}.3f", speed);
        UpdatePlaybackSpeed(speed);
    }
    if (newMode != config_.initMode || DoubleUtils::IsNotEqual(newRatio, config_.retentionRatio)) {
        ApplyNewStrategy(newMode, newRatio);
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
    noRefFrameMap_.clear();
    lastValidMVStats_ = {};
    hasValidMVStats_ = false;
#ifdef SFD_DEBUG
    if (dumper_) {
        dumper_->DumpToFile();
    }
#endif
    AVCODEC_LOGI_WITH_TAG("SFD flushed (counters and trackers reset)");
}

void SmartFluencyDecoding::SetInputBuffer(uint32_t index, const std::shared_ptr<AVBuffer>& buffer)
{
    AVCODEC_LOGD_WITH_TAG("SetInputBuffer: index %{public}u, buffer valid: %{public}d",
        index, (buffer != nullptr));
    std::lock_guard<std::mutex> lock(configMutex_);
    if (buffer) {
        inBufferMap_[index] = buffer;
    }
}

void SmartFluencyDecoding::SetCallbacks(AsyncDropInputCallback inCb, AsyncDropOutputCallback outCb)
{
    AVCODEC_LOGD_WITH_TAG("SetCallbacks: inCb valid: %{public}d, outCb valid: %{public}d",
        (inCb != nullptr), (outCb != nullptr));
    std::lock_guard<std::mutex> lock(configMutex_);
    asyncDropInputCb_ = inCb;
    asyncDropOutputCb_ = outCb;
}

bool SmartFluencyDecoding::InitializeWithCsd(const std::vector<uint8_t>& csdData)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(isInitialized_, false, "InitializeWithCsd skipped, not initialized");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!csdData.empty(), false, "InitializeWithCsd failed, csdData is empty");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(csdData.size() <= static_cast<size_t>(INT32_MAX), false,
        "InitializeWithCsd failed, csdData size (=%{public}zu) exceeds INT32_MAX", csdData.size());
    std::shared_ptr<Media::AVAllocator> allocator =
        Media::AVAllocatorFactory::CreateSharedAllocator(Media::MEMORY_READ_WRITE);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(allocator != nullptr, false, "Failed to create shared allocator for replay");
    auto tmpBuffer = Media::AVBuffer::CreateAVBuffer(allocator, static_cast<int32_t>(csdData.size()));
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(tmpBuffer != nullptr && tmpBuffer->memory_ != nullptr, false,
        "Failed to create AVBuffer or memory is null");
    int32_t capacity = tmpBuffer->memory_->GetCapacity();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(capacity >= static_cast<int32_t>(csdData.size()), false,
        "Created buffer capacity (=%{public}d) is smaller than required (=%{public}zu)",
        capacity, csdData.size());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(memcpy_s(tmpBuffer->memory_->GetAddr(), capacity,
        csdData.data(), csdData.size()) == EOK, false, "Failed to copy CSD data");
    tmpBuffer->memory_->SetSize(csdData.size());
    tmpBuffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    csdBuffer_ = tmpBuffer;
    if (naluAnalyzer_) {
        (void)naluAnalyzer_->IsNonReferenceFrame(csdBuffer_);
        AVCODEC_LOGI_WITH_TAG("SFD Mid-stream enabled: Replayed CSD data processed by NaluAnalyzer, size=%{public}zu",
            csdData.size());
        return true;
    }
    AVCODEC_LOGD_WITH_TAG("naluAnalyzer_ not instantiated yet, caching CSD buffer (size=%{public}zu).",
        csdData.size());
    return false;
}

void SmartFluencyDecoding::ExecutePreDrop(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "ExecutePreDrop: buffer is null for index %{public}u", index);
    AVCODEC_LOGI_WITH_TAG("[Pre-Drop] Executing drop for index %{public}u, pts %{public}" PRId64
        ", flag %{public}u", index, buffer->pts_, buffer->flag_);
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer->flag_ != AVCODEC_BUFFER_FLAG_CODEC_DATA,
        "ExecutePreDrop: skip CODEC_DATA buffer");
    dropSyncCoordinator_->RecordPreDroppedFrame(buffer->pts_);
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
        caps.targetRatio = strategy_->GetRatio();
        caps.isPassthrough = strategy_->IsPassthrough();
        caps.requiresNalu = strategy_->RequiresNaluAnalysis();
        caps.requiresMv = strategy_->RequiresMvAnalysis();
    }
    return buffer;
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
    dumperCtx.actualRetentionRatio = dropSyncCoordinator_->GetActualRetentionRatio();
    dumperCtx.reason = res.reason;
    dumper_->RecordPreDecode(dumperCtx);
#endif
}


void SmartFluencyDecoding::RecordPostDecodeDump(
    int64_t pts, uint32_t preDropCount, const PostDecResult& res,
    const MVStats& mvStats, const PostDecCost& cost)
{
#ifdef SFD_DEBUG
    if (!dumper_) {
        return;
    }
    int64_t dumperStart = GetCurrentSysTimeUs();
    SmartFluencyDecodingDumper::PostDecodeRecordCtx dumperCtx;
    dumperCtx.pts = pts;
    dumperCtx.isRetained = res.shouldRetain;
    dumperCtx.preDropCount = preDropCount;
    dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
    dumperCtx.postDecCostUs = dumperStart - cost.startUs;
    dumperCtx.readIndex = dropSyncCoordinator_->GetReadIndex();
    dumperCtx.actualRetentionRatio = dropSyncCoordinator_->GetActualRetentionRatio();
    dumperCtx.reason = res.reason;
    dumperCtx.mvStats = mvStats;
    dumperCtx.copyMvCostUs = cost.copyMvCostUs;
    dumperCtx.analyzeMvCostUs = cost.analyzeMvCostUs;
    dumperCtx.updateDumperCostUs = dumper_->GetLastOverhead();
    dumper_->RecordPostDecode(dumperCtx);
    dumper_->SetLastOverhead(GetCurrentSysTimeUs() - dumperStart);
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
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
        AVCODEC_LOGI_WITH_TAG("ignore eos frame");
        return true;
    }
    PreDecResult res;
    if (naluAnalyzer_ && caps.requiresNalu) {
#ifdef SFD_DEBUG
        int64_t naluStartUs = GetCurrentSysTimeUs();
#endif
        res.isNonRef = naluAnalyzer_->IsNonReferenceFrame(buffer);
        SetNoRefFrameMap(buffer->pts_, res.isNonRef);
#ifdef SFD_DEBUG
        res.naluCostUs = GetCurrentSysTimeUs() - naluStartUs;
#endif
        if (res.isNonRef) {
            double currentRetentionRatio = dropSyncCoordinator_->GetActualRetentionRatio();
            if (DoubleUtils::IsGreaterOrEqual(caps.targetRatio, 1.0)) {
                res.reason = SfdDecisionReason::PRE_RETAIN_TARGET_FULL;
            } else if (DoubleUtils::IsLessOrEqual(currentRetentionRatio, caps.targetRatio)) {
                res.reason = SfdDecisionReason::PRE_RETAIN_RATIO_SATISFIED;
            } else {
                res.shouldRetain = false;
                res.reason = SfdDecisionReason::PRE_DROP_RATIO_UNSATISFIED;
            }
        }
    } else if (caps.isPassthrough) {
        res.reason = SfdDecisionReason::PRE_RETAIN_TARGET_FULL;
    }

    if (!res.shouldRetain) {
        ExecutePreDrop(index, buffer);
    }
#ifdef SFD_DEBUG
    RecordPreDecodeDump(buffer, res, startUs);
#endif
    AVCODEC_LOGD_WITH_TAG("MakePreDecodeDecision: index %{public}u, pts %{public}" PRId64 ", isNonRef %{public}d"
        ", shouldRetain %{public}d", index, buffer->pts_, res.isNonRef, res.shouldRetain);
    return res.shouldRetain;
}

void SmartFluencyDecoding::ExecutePostDrop(uint32_t index)
{
    AVCODEC_LOGD_WITH_TAG("ExecutePostDrop: Dropping frame at index %{public}u", index);
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

bool SmartFluencyDecoding::HandlePostDecodePassthrough(int64_t pts, uint32_t preDropCount, const PostDecCost& cost)
{
    UpdateRetentionFeedback(true);
#ifdef SFD_DEBUG
    PostDecResult fastRes {true, 1.0, SfdDecisionReason::POST_RETAIN_FULL};
    RecordPostDecodeDump(pts, preDropCount, fastRes, MVStats(), cost);
#endif
    return true;
}

void SmartFluencyDecoding::UpdateRetentionFeedback(bool shouldRetain)
{
    std::lock_guard<std::mutex> lock(configMutex_);
    if (dropSyncCoordinator_) {
        dropSyncCoordinator_->UpdateFeedback(shouldRetain);
    }
}

bool SmartFluencyDecoding::MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer)
{
    PostDecCost cost;
#ifdef SFD_DEBUG
    cost.startUs = GetCurrentSysTimeUs();
#endif
    if (!isInitialized_ || !dropSyncCoordinator_) {
        return true;
    }
    uint32_t preDropCount = dropSyncCoordinator_->GetAndConsumePreDroppedCount(pts);
    StrategyCaps caps;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        if (!strategy_) {
            return true;
        }
        caps.isPassthrough = strategy_->IsPassthrough();
        caps.requiresMv = strategy_->RequiresMvAnalysis();
    }
    if (caps.isPassthrough) {
        return HandlePostDecodePassthrough(pts, preDropCount, cost);
    }

    MVStats mvStats = ExtractMVStats(caps, pts, cost, surfaceBuffer);
    PostDecResult res;
    FrameRetentionContext ctx { preDropCount, caps.requiresMv ? &mvStats : nullptr };
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        if (strategy_) {
            res.shouldRetain = strategy_->MakeRetentionDecision(ctx);
            res.ratio = strategy_->GetRatio();
            res.reason = strategy_->GetLastDecisionReason();
        }
    }
    UpdateRetentionFeedback(res.shouldRetain);
    AVCODEC_LOGD_WITH_TAG("Strategy decision: shouldRetain %{public}d, ratio %{public}.3f "
        "(index:%{public}u, pts:%{public}" PRId64 ")", res.shouldRetain, res.ratio, index, pts);
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
    CHECK_AND_RETURN_LOG_WITH_TAG(mvAnalyzer_ != nullptr, "mvAnalyzer_ is null, skipping MV analysis");
    CHECK_AND_RETURN_LOG_WITH_TAG(surfaceBuffer != nullptr, "surfaceBuffer is null");
#ifdef SFD_DEBUG
    int64_t copyStart = GetCurrentSysTimeUs();
#endif
    using namespace HDI::Display::Graphic::Common;
    std::vector<uint8_t> vec;
    V2_0::BlobDataType data;
    int32_t ret = surfaceBuffer->GetMetadata(V2_0::ATTRKEY_VIDEO_DECODER_MV, vec);
    CHECK_AND_RETURN_LOG_WITH_TAG(ret == GSERROR_OK && vec.size() == sizeof(V2_0::BlobDataType),
        "getMetadata failed or size mismatch (ret:%{public}d, size:%{public}zu)", ret, vec.size());
    CHECK_AND_RETURN_LOG_WITH_TAG(memcpy_s(&data, sizeof(V2_0::BlobDataType),
        vec.data(), vec.size()) == EOK, "memcpy_s failed");
#ifdef SFD_DEBUG
    int64_t copyEnd = GetCurrentSysTimeUs();
    copyCostUs = copyEnd - copyStart;
    int64_t analyzeStart = copyEnd;
#endif
    uint8_t* input = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(data.vaddr + data.offset));
    mvStats = mvAnalyzer_->Analyze(input, data.length);
#ifdef SFD_DEBUG
    analyzeCostUs = GetCurrentSysTimeUs() - analyzeStart;
#endif
}

void SmartFluencyDecoding::EnsureNaluAnalyzer()
{
    std::unique_lock<std::mutex> lock(configMutex_);
    if (naluAnalyzer_) {
        return;
    }
    AVCODEC_LOGI_WITH_TAG("Creating new NaluAnalyzer for codecType %{public}d",
        static_cast<int32_t>(config_.codecType));
    auto analyzer = NaluAnalyzerFactory::CreateNaluAnalyzer(config_.codecType);
    CHECK_AND_RETURN_LOG_WITH_TAG(analyzer != nullptr, "No NALU analyzer available for codecType %{public}d",
        static_cast<int32_t>(config_.codecType));
    analyzer->SetAuxInfoCallback([this](const StreamAuxInfo& info) {
        std::lock_guard<std::mutex> cbLock(this->configMutex_);
        AVCODEC_LOGI_WITH_TAG("Received StreamAuxInfo: directSpatialMvPredFlag=%{public}u, "
            "direct8x8InferenceFlag=%{public}u, frameMbsOnlyFlag=%{public}d, ctuSize=%{public}u",
            info.directSpatialMvPredFlag, info.direct8x8InferenceFlag, info.frameMbsOnlyFlag, info.ctuSize);
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
        AVCODEC_LOGD_WITH_TAG("No cached CSD buffer to replay.");
        return;
    }
    AVCODEC_LOGI_WITH_TAG("Replaying cached CSD buffer to activate newly instantiated NaluAnalyzer");
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
    AVCODEC_LOGI_WITH_TAG("EnsureMvAnalyzer: Creating new MvAnalyzer for codecType %{public}d",
        static_cast<int32_t>(config_.codecType));
    mvAnalyzer_ = MvAnalyzerFactory::CreateMvAnalyzer(config_.codecType);
    CHECK_AND_RETURN_LOG_WITH_TAG(mvAnalyzer_ != nullptr,
        "Failed to create MV parser for codec type: %{public}d", static_cast<int>(config_.codecType));
    mvAnalyzer_->SetVideoInfo(config_.width, config_.height);
}

bool SmartFluencyDecoding::IsRatioValid(double ratio)
{
    CHECK_AND_RETURN_RET_LOG(DoubleUtils::IsGreaterOrEqual(ratio, MIN_RETENTION_RATIO), false,
        "Retention ratio invalid: value=%{public}.3f (must be >= %{public}.3f)", ratio, MIN_RETENTION_RATIO);
    CHECK_AND_RETURN_RET_LOG(DoubleUtils::IsLessOrEqual(ratio, MAX_RETENTION_RATIO), false,
        "Retention ratio invalid: value=%{public}.3f (must be <= %{public}.3f)", ratio, MAX_RETENTION_RATIO);
    return true;
}

void SmartFluencyDecoding::SetNoRefFrameMap(int64_t pts, bool isNonRef)
{
    std::lock_guard<std::mutex> lock(noRefFrameMapMutex_);
    noRefFrameMap_[pts] = isNonRef;
}

bool SmartFluencyDecoding::GetNoRefFrameFromMap(int64_t pts)
{
    std::lock_guard<std::mutex> lock(noRefFrameMapMutex_);
    auto it = noRefFrameMap_.find(pts);
    bool isNonRef = false;
    if (it != noRefFrameMap_.end()) {
        isNonRef = it->second;
        noRefFrameMap_.erase(noRefFrameMap_.begin(), std::next(it));
    } else {
        AVCODEC_LOGE_WITH_TAG("refMap can't find pts: %{public}" PRId64 "", pts);
    }

    return isNonRef;
}

MVStats SmartFluencyDecoding::ExtractMVStats(StrategyCaps caps, int64_t pts,
                                             PostDecCost& cost, const sptr<SurfaceBuffer> surfaceBuffer)
{
    if (!caps.requiresMv || surfaceBuffer == nullptr) {
        return MVStats{};
    }

    bool isNonRef = GetNoRefFrameFromMap(pts);
    if (isNonRef) {
        return hasValidMVStats_ ? lastValidMVStats_ : MVStats{};
    }

    MVStats mvStats{};
    ParseMVData(surfaceBuffer, mvStats, cost.copyMvCostUs, cost.analyzeMvCostUs);
    if (mvStats.validBlocks > 0) {
        lastValidMVStats_ = mvStats;
        hasValidMVStats_ = true;
    }

    return mvStats;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
