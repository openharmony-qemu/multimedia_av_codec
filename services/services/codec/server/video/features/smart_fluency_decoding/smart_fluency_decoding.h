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

#ifndef SMART_FLUENCY_DECODING_H
#define SMART_FLUENCY_DECODING_H

#include "avcodec_dfx_component.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include "async_drop_dispatcher.h"
#include "drop_sync_coordinator.h"
#include "strategies/retention_strategy.h"
#include "meta/format.h"
#include "analyzers/mv_analyzer.h"
#include "analyzers/nalu_analyzer.h"
#ifdef SFD_DEBUG
#include "smart_fluency_decoding_dumper.h"
#endif
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class SmartFluencyDecoding : public AVCodecDfxComponent {
public:
    static bool IsRatioValid(double ratio);
    static void ParseRetentionConfig(
        const Media::Format& format, RetentionStrategyType& mode, std::optional<double>& ratio);

    SmartFluencyDecoding() = default;
    ~SmartFluencyDecoding() = default;

    int32_t Initialize(const SFDConfig& config);
    void UpdatePlaybackSpeed(double speed);
    int32_t UpdateDynamicConfig(const Media::Format& format);
    void SetInputBuffer(uint32_t index, const std::shared_ptr<AVBuffer>& buffer);

    using AsyncDropInputCallback = std::function<void(uint32_t index, std::shared_ptr<AVBuffer> buffer)>;
    using AsyncDropOutputCallback = std::function<void(uint32_t index)>;
    void SetCallbacks(AsyncDropInputCallback inCb, AsyncDropOutputCallback outCb);
    bool InitializeWithCsd(const std::vector<uint8_t>& csdData);
    bool MakePreDecodeDecision(uint32_t index);
    bool MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer);
    void Flush();
    void OnDecInfoUpdate(double sourceFps, double speed);
    
private:
    struct StrategyCaps {
        bool isPassthrough{false};
        bool requiresNalu{true};
        bool requiresMv{false};
        double targetRatio{1.0};
    };

    struct PreDecResult {
        bool shouldRetain{true};
        bool isNonRef{false};
        SfdDecisionReason reason{SfdDecisionReason::UNSPECIFIED};
        int64_t naluCostUs{-1};
    };

    struct PostDecResult {
        bool shouldRetain{true};
        double ratio{1.0};
        SfdDecisionReason reason{SfdDecisionReason::UNSPECIFIED};
    };

    struct PostDecCost {
        int64_t startUs{-1};
        int64_t copyMvCostUs{-1};
        int64_t analyzeMvCostUs{-1};
    };

    std::shared_ptr<AVBuffer> FetchInputBufferAndCapabilities(uint32_t index, StrategyCaps& caps);
    MVStats ExtractMVStats(StrategyCaps caps, int64_t pts, PostDecCost& cost, const sptr<SurfaceBuffer> surfaceBuffer);
    void RecordPreDecodeDump(const std::shared_ptr<AVBuffer>& buffer, const PreDecResult& res, int64_t startUs);
    void RecordPostDecodeDump(int64_t pts, uint32_t preDropCount, const PostDecResult& res,
                              const MVStats& mvStats, const PostDecCost& cost);
    int32_t InitializeInternal();
    int64_t GetCurrentSysTimeMs();
    void ParseMVData(const sptr<SurfaceBuffer> surfaceBuffer, MVStats& mvStats,
                     int64_t& copyCostUs, int64_t& analyzeCostUs);
    void EnsureNaluAnalyzer();
    void EnsureMvAnalyzer();
    void ExecutePreDrop(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void ExecutePostDrop(uint32_t index);
    bool HandlePostDecodePassthrough(int64_t pts, uint32_t preDropCount, const PostDecCost& cost);
    void UpdateRetentionFeedback(bool shouldRetain, double speed);
    void ApplyNewStrategy(RetentionStrategyType newMode, const std::optional<double>& newRatio);
    void SetNoRefFrameMap(int64_t pts, bool isNonRef);
    bool GetNoRefFrameFromMap(int64_t pts);

    mutable std::mutex configMutex_;
    std::unique_ptr<DropSyncCoordinator> dropSyncCoordinator_{nullptr};
    std::unique_ptr<AsyncDropDispatcher> asyncDispatcher_{nullptr};
    std::unique_ptr<IRetentionStrategy> strategy_{nullptr};
    std::unique_ptr<IMvAnalyzer> mvAnalyzer_{nullptr};
    std::unique_ptr<INaluAnalyzer> naluAnalyzer_{nullptr};
#ifdef SFD_DEBUG
    std::unique_ptr<SmartFluencyDecodingDumper> dumper_{nullptr};
#endif
    MVStats lastValidMVStats_{};
    std::map<int64_t, bool> noRefFrameMap_;
    mutable std::mutex noRefFrameMapMutex_;
    bool hasValidMVStats_{false};
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> inBufferMap_;
    std::shared_ptr<AVBuffer> csdBuffer_{nullptr};
    SFDConfig config_{};
    bool isInitialized_{false};
    double currentPlaybackSpeed_{1.0};
    AsyncDropInputCallback asyncDropInputCb_{nullptr};
    AsyncDropOutputCallback asyncDropOutputCb_{nullptr};
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SMART_FLUENCY_DECODING_H