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

#if defined(__cplusplus) && !defined(NALU_ANALYZER_PLUGIN_COMPILE)
#include <functional>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

enum class SfdDecisionReason : int32_t {
    UNSPECIFIED = 0,
    
    // --- Pre-Decode 决策原因 ---
    PRE_RETAIN_REFERENCE = 1,        // 保留：当前帧是参考帧
    PRE_RETAIN_TARGET_FULL = 2,      // 保留：目标丢帧率极低 (如 FULL 模式)
    PRE_RETAIN_EMA_SATISFIED = 3,    // 保留：当前 EMA 丢帧率已达标
    PRE_DROP_EMA_UNSATISFIED = 4,    // 丢弃：当前 EMA 不足，需丢弃以补偿丢帧率

    // --- Post-Decode 决策原因 ---
    POST_BYPASSED = 10,              // 跳过：该帧在解码前(Pre-Decode)已被丢弃
    POST_RETAIN_FULL = 11,           // 保留：FULL 全量保留策略
    POST_RETAIN_FIXED_ACC = 12,      // 保留：FixedRatio 策略，累加器达到阈值
    POST_DROP_FIXED_ACC = 13,        // 丢弃：FixedRatio 策略，累加器未达阈值
    POST_RETAIN_ADAPTIVE_SPEED = 14, // 保留：Adaptive 策略，当前播放倍速较低无需降帧
    POST_RETAIN_ADAPTIVE_DT = 15,    // 保留：Adaptive 策略，决策树(DT)评估为无需丢弃
    POST_RETAIN_ADAPTIVE_MAX_DROP = 16, // 保留：Adaptive 策略，触发最大连续丢帧限制强制放行
    POST_DROP_ADAPTIVE_DT = 17,      // 丢弃：Adaptive 策略，决策树评估可丢且未达限制
    POST_RETAIN_AUTO_FALLBACK = 18   // 保留：AutoFallback 策略，处于探测期或保底间隔期
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

#endif /* defined(__cplusplus) && !defined(NALU_ANALYZER_PLUGIN_COMPILE) */
#endif // SMART_FLUENCY_DECODING_TYPES_H
