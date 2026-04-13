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
    if (config.width <= 0 || config.height <= 0) {
        AVCODEC_LOGE("Invalid video dimensions: %{public}dx%{public}d", config.width, config.height);
        return AVCS_ERR_INVALID_VAL;
    }
    if (config.codecType == SFDCodecType::INVALID) {
        AVCODEC_LOGE("Unsupported codec type");
        return AVCS_ERR_INVALID_VAL;
    }

    config_ = config;

    int32_t ret = initializeInternal();
    if (ret != 0) {
        AVCODEC_LOGE("Failed to initialize internal components");
        return AVCS_ERR_UNKNOWN;
    }

    isInitialized_ = true;
    AVCODEC_LOGI("SmartFluencyDecoding initialized: %{public}dx%{public}d, codecType=%{public}d, mode=%{public}d",
                  config_.width, config_.height, static_cast<int32_t>(config_.codecType), static_cast<int32_t>(config_.initMode));
    
    return AVCS_ERR_OK;
}

int32_t SmartFluencyDecoding::initializeInternal()
{
    // Lazy Initialization
    if (config_.initMode == RetentionStrategyType::AUTO_RATIO || 
        config_.initMode == RetentionStrategyType::FIXED_RATIO || 
        config_.initMode == RetentionStrategyType::ADAPTIVE) {
        EnsureNaluAnalyzer();
    }
    if (config_.initMode == RetentionStrategyType::ADAPTIVE) {
        EnsureMvAnalyzer();
    }
    
    strategy_ = RetentionStrategyFactory::CreateStrategy(config_.initMode, config_.retentionRatio);
    if (!strategy_) {
        AVCODEC_LOGE("Failed to create strategy");
        return -1;
    }

    dropSyncCoordinator_ = std::make_unique<DropSyncCoordinator>();
    asyncDispatcher_ = std::make_unique<AsyncDropDispatcher>();
    dumper_ = std::make_unique<SfdDumper>();
    dumper_->Init(config_.codecType, config_.width, config_.height, config_.initMode);
    if (config_.retentionRatio.has_value()) {
        dumper_->UpdateMode(config_.initMode, config_.retentionRatio.value());
    }
    
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
        if (dumper_) {
            dumper_->UpdateSpeed(speed);
        }
        AVCODEC_LOGI("SFD Playback speed dynamically updated to: %{public}.2f", speed);
    }
}

void SmartFluencyDecoding::ParseDynamicFormat(const Media::Format& format, RetentionStrategyType& outMode, std::optional<double>& outRatio) const
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
            // 按需挂载底层组件（如果发生模式切换，可能有组件尚未初始化）
            // 注意：必须在获取 configMutex_ 之前调用，因为内部回放 CSD 会触发回调并尝试获取该锁
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
            config_.initMode = newMode;
            config_.retentionRatio = newRatio;
            if (dumper_) {
                dumper_->UpdateMode(newMode, newRatio.has_value() ? newRatio.value() : DEFAULT_PLAYBACK_SPEED);
            }
            AVCODEC_LOGI("Dynamic SFD mode updated from %{public}d to %{public}d", 
                         static_cast<int32_t>(config_.initMode), static_cast<int32_t>(newMode));
        } else {
            AVCODEC_LOGW("Failed to dynamically update SFD mode to %{public}d", static_cast<int32_t>(newMode));
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
    
    if (dumper_) {
        dumper_->DumpToFile();
    }
    
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
        return false;
    }

    // 防御性编程：只处理带有 CODEC_DATA 标志的 Buffer
    if (!(csdBuffer->flag_ & AVCODEC_BUFFER_FLAG_CODEC_DATA)) {
        AVCODEC_LOGW("Buffer passed to InitializeWithCsd does not have CODEC_DATA flag.");
        return false;
    }

    // 缓存 CSD，以便后续从 FULL 切换到其他模式时重新激活探针
    csdBuffer_ = csdBuffer;

    if (naluAnalyzer_) {
        // 让底层探针去解析 CSD 并触发 AuxInfoCallback 更新 CTU Size 等信息。
        (void)naluAnalyzer_->IsNonReferenceFrame(csdBuffer);
        AVCODEC_LOGI("CSD data processed by NaluAnalyzer.");
        return true;
    }

    return false;
}

bool SmartFluencyDecoding::MakePreDecodeDecision(uint32_t index)
{
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

    bool shouldDrop = false;
    bool isNonRef = false;
    if (naluAnalyzer_ && naluAnalyzer_->IsNonReferenceFrame(buffer)) {
        isNonRef = true;
        
        // 读取跨线程反馈面板
        double currentEma = dropSyncCoordinator_->GetCurrentEma();
        double targetRetentionRatio = dropSyncCoordinator_->GetTargetRetentionRatio();
        double targetDropRatio = 1.0 - targetRetentionRatio;

        if (currentEma >= targetDropRatio && targetDropRatio > RETENTION_RATIO_MIN_DIFF) {
            shouldDrop = false; 
        } else {
            shouldDrop = true;
        }
    }

    if (shouldDrop) {
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
    } else {
        totalInputFrames_++;
    }

    if (dumper_) {
        SfdDumper::PreDecodeRecordCtx dumperCtx;
        dumperCtx.pts = buffer->pts_;
        dumperCtx.dts = buffer->dts_;
        dumperCtx.isNonRef = isNonRef;
        dumperCtx.flags = buffer->flag_;
        dumperCtx.dropped = shouldDrop;
        dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
        dumper_->RecordPreDecode(dumperCtx);
    }

    // 后置修改点：返回是否保留。true 表示保留（放行），false 表示丢弃。
    return !shouldDrop;
}

bool SmartFluencyDecoding::MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer)
{
    if (!isInitialized_ || !strategy_ || !dropSyncCoordinator_) {
        return true;
    }

    int64_t currentPts = pts;

    int32_t preDroppedCount = dropSyncCoordinator_->GetAndConsumePreDroppedCount(currentPts);

    MVStats currentMvStats;
    parseMVData(surfaceBuffer, currentMvStats);

    FrameRetentionContext ctx;
    ctx.ptsMs = currentPts;
    ctx.sysTimeMs = GetCurrentSysTimeMs(); 
    ctx.preDroppedCount = preDroppedCount;
    ctx.mvStats = &currentMvStats;

    bool shouldRetain = false;
    double currentRetentionRatio = 1.0;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        shouldRetain = strategy_->MakeRetentionDecision(ctx);
        currentRetentionRatio = strategy_->GetCurrentRetentionRatio();
    }

    // 更新 EMA 反馈状态
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        dropSyncCoordinator_->UpdateFeedback(preDroppedCount, !shouldRetain, currentRetentionRatio, currentPlaybackSpeed_);
    }

    totalOutputFrames_++;
    if (!shouldRetain) {
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

    if (dumper_) {
        SfdDumper::PostDecodeRecordCtx dumperCtx;
        dumperCtx.pts = currentPts;
        dumperCtx.dropped = !shouldRetain;
        dumperCtx.preDropCount = preDroppedCount;
        dumperCtx.sysTimeMs = GetCurrentSysTimeMs();
        dumper_->RecordPostDecode(dumperCtx);
    }

    return shouldRetain;
}

int64_t SmartFluencyDecoding::GetCurrentSysTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void SmartFluencyDecoding::parseMVData(const sptr<SurfaceBuffer> surfaceBuffer,
                                         MVStats& mvStats)
{
    mvStats = MVStats(); // 使用 DTO 进行重置

    if (!mvAnalyzer_) {
        AVCODEC_LOGE("parse MV Data failed! mvParser don't exist");
        return;
    }
    if (!surfaceBuffer) {
        AVCODEC_LOGE("parse MV Data failed! surfaceBuffer don't exist");
    }

    using namespace HDI::Display::Graphic::Common;
    std::vector<uint8_t> vec;
    V2_0::BlobDataType data;
    int32_t ret = surfaceBuffer->GetMetadata(V2_0::ATTRKEY_VIDEO_DECODER_MV, vec);
    ret = memcpy_s(&data, sizeof(V2_0::BlobDataType), vec.data(), vec.size());
    uint8_t* input = (uint8_t*)(data.vaddr + data.offset);

    size_t size = mvAnalyzer_->GetMvDataSize();
    if (size > 0) {
        mvStats = mvAnalyzer_->parse(input, size);
    }
}

void SmartFluencyDecoding::GetStatistics(uint32_t& totalFrames,
                                           uint32_t& droppedFrames) const
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
        if (this->dumper_) {
            this->dumper_->UpdateCtuSize(info.ctuSize);
        }
    });
    
    // 安全赋值，让其他线程可以看到完整的构造对象
    naluAnalyzer_ = std::move(analyzer);

    // 如果是在中途（例如从 FULL 模式切换）才实例化的探针，尝试回放 CSD 头信息以激活它
    if (!csdBuffer_) {
        return;
    }

    AVCODEC_LOGI("Replaying cached CSD buffer to activate newly instantiated NaluAnalyzer");
    auto cachedCsd = csdBuffer_;
    // 解锁以防止回调时发生死锁
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
    if (!mvAnalyzer_) {
        AVCODEC_LOGE("Failed to create MV parser for codec type: %{public}d", static_cast<int>(config_.codecType));
        return;
    }
    
    // Pass video resolution to the parser
    mvAnalyzer_->SetVideoInfo(config_.width, config_.height);
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

