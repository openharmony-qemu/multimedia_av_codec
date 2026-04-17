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
    if (config_.initMode == RetentionStrategyType::AUTO_RATIO ||
        config_.initMode == RetentionStrategyType::FIXED_RATIO ||
        config_.initMode == RetentionStrategyType::ADAPTIVE) {
        EnsureNaluAnalyzer();
    }
    if (config_.initMode == RetentionStrategyType::ADAPTIVE) {
        EnsureMvAnalyzer();
    }
    strategy_ = RetentionStrategyFactory::CreateStrategy(config_.initMode, config_.retentionRatio);
    CHECK_AND_RETURN_RET_LOG(strategy_ != nullptr, AVCS_ERR_UNKNOWN, "Failed to create strategy");
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

void SmartFluencyDecoding::ParseDynamicFormat(
    const Media::Format& format, RetentionStrategyType& outMode, std::optional<double>& outRatio) const
{
    int32_t retentionModeInt = -1;
    if (format.GetIntValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, retentionModeInt)) {
        if (retentionModeInt == static_cast<int32_t>(Media::Plugins::FrameRetentionMode::FULL)) {
            outMode = RetentionStrategyType::FULL;
        } else if (retentionModeInt == static_cast<int32_t>(Media::Plugins::FrameRetentionMode::ADAPTIVE)) {
            outMode = RetentionStrategyType::ADAPTIVE;
        } else if (retentionModeInt == static_cast<int32_t>(Media::Plugins::FrameRetentionMode::UNIFORM)) {
            outMode = outRatio.has_value() ? RetentionStrategyType::FIXED_RATIO : RetentionStrategyType::AUTO_RATIO;
        }
    }
    double ratio = 0.0;
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, ratio)) {
        outRatio = ratio;
        if (outMode == RetentionStrategyType::AUTO_RATIO || outMode == RetentionStrategyType::FIXED_RATIO) {
            outMode = RetentionStrategyType::FIXED_RATIO;
        }
    }
}

int32_t SmartFluencyDecoding::UpdateDynamicConfig(const Media::Format& format)
{
    bool needUpdateStrategy = false;
    RetentionStrategyType newMode = config_.initMode;
    std::optional<double> newRatio = config_.retentionRatio;
    ParseDynamicFormat(format, newMode, newRatio);
    if (newMode != config_.initMode || newRatio != config_.retentionRatio) {
        needUpdateStrategy = true;
    }
    double speed = DEFAULT_PLAYBACK_SPEED;
    if (format.GetDoubleValue(Media::Tag::VIDEO_DECODER_SPEED, speed)) {
        UpdatePlaybackSpeed(speed);
    }
    if (needUpdateStrategy) {
        auto newStrategy = RetentionStrategyFactory::CreateStrategy(newMode, newRatio);
        if (newStrategy) {
            if (newMode == RetentionStrategyType::AUTO_RATIO ||
                newMode == RetentionStrategyType::FIXED_RATIO ||
                newMode == RetentionStrategyType::ADAPTIVE) {
                EnsureNaluAnalyzer();
            }
            if (newMode == RetentionStrategyType::ADAPTIVE) {
                EnsureMvAnalyzer();
            }
            std::lock_guard<std::mutex> lock(configMutex_);
            strategy_ = std::move(newStrategy);
            AVCODEC_LOGE("CSD_DEBUG: SFD mode updated from %{public}d to %{public}d",
                         static_cast<int32_t>(config_.initMode), static_cast<int32_t>(newMode));
            config_.initMode = newMode;
            config_.retentionRatio = newRatio;
#ifdef SFD_DEBUG
            if (dumper_) {
                dumper_->UpdateMode(newMode, newRatio.has_value() ? newRatio.value() : DEFAULT_PLAYBACK_SPEED);
            }
#endif
        } else {
            AVCODEC_LOGE("CSD_DEBUG: Failed to dynamically update SFD mode to %{public}d", static_cast<int32_t>(newMode));
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
    std::lock_guard<std::mutex> lock(configMutex_);
    if (buffer) {
        inBufferMap_[index] = buffer;
    }
}

void SmartFluencyDecoding::SetCallbacks(AsyncDropInputCallback inCb, AsyncDropOutputCallback outCb)
{
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
    dropSyncCoordinator_->RecordPreDroppedFrame(buffer->pts_);
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

bool SmartFluencyDecoding::MakePreDecodeDecision(uint32_t index)
{
#ifdef SFD_DEBUG
    int64_t startUs = GetCurrentSysTimeUs(); // 开始计时
#endif

    std::shared_ptr<AVBuffer> buffer = nullptr;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        auto it = inBufferMap_.find(index);
        if (it != inBufferMap_.end()) {
            buffer = it->second;
            inBufferMap_.erase(it);
        }
    }
    if (!isInitialized_ || !buffer || !dropSyncCoordinator_) {
        return true;
    }
    bool shouldRetain = true;
    bool isNonRef = false;
    if (naluAnalyzer_ && naluAnalyzer_->IsNonReferenceFrame(buffer)) {
        isNonRef = true;
        double currentEma = dropSyncCoordinator_->GetCurrentEma();
        double targetRetentionRatio = dropSyncCoordinator_->GetTargetRetentionRatio();
        double targetDropRatio = 1.0 - targetRetentionRatio;
        shouldRetain = currentEma >= targetDropRatio && targetDropRatio > RETENTION_RATIO_MIN_DIFF;
    }
    if (shouldRetain) {
        totalInputFrames_++;
    } else {
        ExecutePreDrop(index, buffer);
    }
#ifdef SFD_DEBUG
    int64_t endUs = GetCurrentSysTimeUs(); // 结束计时
#endif

#ifdef SFD_DEBUG
    if (dumper_) {
        SmartFluencyDecodingDumper::PreDecodeRecordCtx dumperCtx;
        dumperCtx.pts = buffer->pts_;
        dumperCtx.dts = buffer->dts_;
        dumperCtx.isNonRef = isNonRef;
        dumperCtx.flags = buffer->flag_;
        dumperCtx.dropped = !shouldRetain;
        dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
        
        // --- 填入新增的观测字段 ---
        dumperCtx.preDecCostUs = endUs - startUs;
        dumperCtx.writeIndex = dropSyncCoordinator_->GetWriteIndex();
        dumperCtx.currentEmaDropRatio = dropSyncCoordinator_->GetCurrentEma();
        
        dumper_->RecordPreDecode(dumperCtx);
    }
#endif
    (void)isNonRef;
    return shouldRetain;
}

void SmartFluencyDecoding::ExecutePostDrop(uint32_t index)
{
    AVCODEC_LOGE("CSD_DEBUG: [Post-Drop] Executing drop for index %{public}u", index);
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
#ifdef SFD_DEBUG
    int64_t startUs = GetCurrentSysTimeUs(); // 开始计时
#endif

    if (!isInitialized_ || !strategy_ || !dropSyncCoordinator_ || !surfaceBuffer) {
        return true;
    }
    int64_t currentPts = pts;
    uint32_t preDroppedCount = dropSyncCoordinator_->GetAndConsumePreDroppedCount(currentPts);
    MVStats currentMvStats;
    ParseMVData(surfaceBuffer, currentMvStats);
    FrameRetentionContext ctx;
    ctx.ptsMs = currentPts;
    ctx.sysTimeMs = GetCurrentSysTimeMs();
    ctx.preDroppedCount = preDroppedCount;
    ctx.mvStats = &currentMvStats;
    bool shouldRetain = true;
    double currentRetentionRatio = 1.0;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        shouldRetain = strategy_->MakeRetentionDecision(ctx);
        currentRetentionRatio = strategy_->GetCurrentRetentionRatio();
    }
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        dropSyncCoordinator_->UpdateFeedback(
            preDroppedCount, !shouldRetain, currentRetentionRatio, currentPlaybackSpeed_);
    }
    totalOutputFrames_++;
    if (!shouldRetain) {
        ExecutePostDrop(index);
    }
    
#ifdef SFD_DEBUG
    int64_t endUs = GetCurrentSysTimeUs(); // 结束计时
#endif

#ifdef SFD_DEBUG
    if (dumper_) {
        SmartFluencyDecodingDumper::PostDecodeRecordCtx dumperCtx;
        dumperCtx.pts = currentPts;
        dumperCtx.dropped = !shouldRetain;
        dumperCtx.preDropCount = preDroppedCount;
        dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
        
        // --- 填入新增的观测字段 ---
        dumperCtx.postDecCostUs = endUs - startUs;
        dumperCtx.readIndex = dropSyncCoordinator_->GetReadIndex();
        dumperCtx.currentEmaDropRatio = dropSyncCoordinator_->GetCurrentEma(); // 记录反馈更新后的最新 Ema
        dumperCtx.mvStats = currentMvStats;
        
        dumper_->RecordPostDecode(dumperCtx);
    }
#endif
    return shouldRetain;
}

int64_t SmartFluencyDecoding::GetCurrentSysTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void SmartFluencyDecoding::ParseMVData(const sptr<SurfaceBuffer> surfaceBuffer, MVStats& mvStats)
{
    mvStats = MVStats();
    if (mvAnalyzer_ == nullptr) {
        return;
    }
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "parse MV Data failed! surfaceBuffer don't exist");
    using namespace HDI::Display::Graphic::Common;
    std::vector<uint8_t> vec;
    V2_0::BlobDataType data;
    int32_t ret = surfaceBuffer->GetMetadata(V2_0::ATTRKEY_VIDEO_DECODER_MV, vec);
    if (ret != GSERROR_OK || vec.size() != sizeof(V2_0::BlobDataType)) {
        AVCODEC_LOGD("getMetadata failed");
        return;
    }
    ret = memcpy_s(&data, sizeof(V2_0::BlobDataType), vec.data(), vec.size());
    if (ret != EOK) {
        AVCODEC_LOGD("memcpy_s failed");
        return;
    }
    uint8_t* input = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(data.vaddr + data.offset));
    mvStats = mvAnalyzer_->Analyze(input);
}

void SmartFluencyDecoding::GetStatistics(uint32_t& totalFrames, uint32_t& droppedFrames) const
{
    totalFrames = totalOutputFrames_.load();
    droppedFrames = droppedOutputFrames_.load();
}

void SmartFluencyDecoding::OnDecodingPerformanceUpdate(double speed, double decFps)
{
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
    mvAnalyzer_ = MvAnalyzerFactory::CreateMvAnalyzer(config_.codecType);
    CHECK_AND_RETURN_LOG(mvAnalyzer_ != nullptr,
        "Failed to create MV parser for codec type: %{public}d", static_cast<int>(config_.codecType));
    mvAnalyzer_->SetVideoInfo(config_.width, config_.height);
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
