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

#ifndef SMART_FLUENCY_DECODING_TYPES_H
#define SMART_FLUENCY_DECODING_TYPES_H

#include <cstdint>

typedef struct {
    uint32_t directSpatialMvPredFlag;
    uint32_t direct8x8InferenceFlag;
    uint32_t frameMbsOnlyFlag;
    uint32_t ctuSize;
} StreamAuxInfo;

typedef struct {
    uint32_t totalBlocks;
    uint32_t validBlocks;
    uint32_t skipBlocks;
    uint32_t zeroMotionBlocks;
    double avgRefDist;
    double perceptualMagnitude;
    double motionConsistency;
    double zeroMotionRatio;
} MVStats;

#if defined(__cplusplus) && !defined(SFD_PLUGIN_COMPILE)
#include <functional>
#include <optional>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

constexpr double MIN_RETENTION_RATIO = 0.01;
constexpr double MAX_RETENTION_RATIO = 1.0;
constexpr double FALLBACK_TARGET_FPS = 30.0;

enum class SfdDecisionReason : int32_t {
    UNSPECIFIED = 0,

    // --- Pre-Decode Decision Reasons ---
    PRE_RETAIN_REFERENCE = 1,               // Retain: Frame is a reference frame
    PRE_RETAIN_TARGET_FULL = 2,             // Retain: Target retention ratio is ~100% (FULL mode)
    PRE_RETAIN_RATIO_SATISFIED = 3,         // Retain: Current retention ratio is within target
    PRE_DROP_RATIO_UNSATISFIED = 4,         // Drop: Current retention ratio is too high, need to drop to compensate

    // --- Post-Decode Decision Reasons ---
    POST_BYPASSED = 10,                     // Bypassed: Frame was already dropped in Pre-Decode phase
    POST_RETAIN_FULL = 11,                  // Retain: Passthrough (FULL) strategy
    POST_RETAIN_FIXED_ACC = 12,             // Retain: FixedRatio strategy, accumulator reached threshold
    POST_DROP_FIXED_ACC = 13,               // Drop: FixedRatio strategy, accumulator below threshold
    POST_RETAIN_ADAPTIVE_SPEED = 14,        // Retain: Adaptive strategy, playback speed is low
    POST_RETAIN_ADAPTIVE_DT = 15,           // Retain: Adaptive strategy, Decision Tree (DT) allows retention
    POST_DROP_ADAPTIVE_DT = 16,             // Drop: Adaptive strategy, DT recommends dropping
    POST_RETAIN_AUTO_FALLBACK = 17,         // Retain: AutoFallback strategy, releasing at safe time interval
    POST_RETAIN_SYNC_FRAME = 18,            // Retain: Output carries the sync-frame flag
};

using OnStreamAuxInfoParsed = std::function<void(const StreamAuxInfo& info)>;

enum class RetentionStrategyType : int32_t {
    INVALID = -1,
    FULL = 0,
    ADAPTIVE = 1,
    FIXED_RATIO = 2,
    AUTO_RATIO = 3
};

enum class SFDCodecType : int32_t {
    INVALID = -1,
    AVC = 0,
    HEVC = 1,
    VVC = 2
};

struct SFDConfig {
    int32_t width{0};
    int32_t height{0};
    SFDCodecType codecType{SFDCodecType::INVALID};
    RetentionStrategyType initMode{RetentionStrategyType::INVALID};
    std::optional<double> retentionRatio{std::nullopt};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif /* defined(__cplusplus) && !defined(SFD_PLUGIN_COMPILE) */
#endif // SMART_FLUENCY_DECODING_TYPES_H
