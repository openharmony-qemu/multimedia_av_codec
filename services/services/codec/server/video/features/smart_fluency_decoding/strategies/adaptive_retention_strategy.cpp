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

#include "adaptive_retention_strategy.h"
#include "double_utils.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AdaptiveStrategy"};
constexpr double MAX_SPEED_FOR_RETENTION = 1.0;
constexpr double MAX_DROPS_MULTIPLIER_HIGH = 5.0;

struct DecisionNode {
    bool isLeaf;
    union {
        struct {
            double threshold;
            int8_t featureIndex;
            int8_t left;
            int8_t right;
        } internal;
        struct {
            int8_t classId;
        } leaf;
    };
    constexpr DecisionNode(double th, int8_t fIdx, int8_t l, int8_t r)
        : isLeaf(false), internal{th, fIdx, l, r} {}
    constexpr explicit DecisionNode(int classId)
        : isLeaf(true), leaf{static_cast<int8_t>(classId)} {}
};
constexpr int NUM_RULES = 15;
constexpr DecisionNode RETENTION_DECISION_RULES[NUM_RULES] = {
    DecisionNode(21.0108261108, 0, 1, 8),  // rule 0
    DecisionNode(6.5326397419, 0, 2, 5),   // rule 1
    DecisionNode(0.6923948228, 1, 3, 4),   // rule 2
    DecisionNode(0),                       // rule 3: leaf
    DecisionNode(0),                       // rule 4: leaf
    DecisionNode(0.4727701843, 1, 6, 7),   // rule 5
    DecisionNode(1),                       // rule 6: leaf
    DecisionNode(1),                       // rule 7: leaf
    DecisionNode(0.5533757210, 1, 9, 12),  // rule 8
    DecisionNode(0.3526516557, 1, 10, 11), // rule 9
    DecisionNode(1),                       // rule 10: leaf
    DecisionNode(1),                       // rule 11: leaf
    DecisionNode(0.9898027778, 1, 13, 14), // rule 12
    DecisionNode(1),                       // rule 13: leaf
    DecisionNode(0),                       // rule 14: leaf
};

class FluencyEvaluator {
public:
    static bool ShouldRetainFrame(double perceptualMagnitude, double motionConsistency)
    {
        double features[2] = {perceptualMagnitude, motionConsistency}; // 2: feature count
        int8_t ruleId = 0;

        while (!RETENTION_DECISION_RULES[ruleId].isLeaf) {
            const auto& rule = RETENTION_DECISION_RULES[ruleId].internal;
            if (OHOS::MediaAVCodec::SFD::DoubleUtils::IsLessOrEqual(features[rule.featureIndex], rule.threshold)) {
                ruleId = rule.left;
            } else {
                ruleId = rule.right;
            }
        }
        return RETENTION_DECISION_RULES[ruleId].leaf.classId != 0;
    }
};

} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

AdaptiveRetentionStrategy::AdaptiveRetentionStrategy()
{
    AVCODEC_LOGD("AdaptiveRetentionStrategy created");
}

bool AdaptiveRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    bool isRetain = true;
    if (DoubleUtils::IsLessOrEqual(currentSpeed_, MAX_SPEED_FOR_RETENTION)) {
        isRetain = true;
        lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_SPEED;
        targetRatio_ = 1.0;
    } else if (ctx.mvStats && ctx.mvStats->validBlocks > 0) {
        double consistency = ctx.mvStats->motionConsistency;
        double magnitude = ctx.mvStats->perceptualMagnitude * currentSpeed_;
        bool shouldRetain = FluencyEvaluator::ShouldRetainFrame(magnitude, consistency);
        if (!shouldRetain) {
            dropCount_++;
            uint32_t maxDrops = static_cast<uint32_t>((currentSpeed_ - 1.0) * MAX_DROPS_MULTIPLIER_HIGH);
            targetRatio_ = 1.0 / (maxDrops + 1.0);
            if (dropCount_ <= maxDrops) {
                isRetain = false;
                lastReason_ = SfdDecisionReason::POST_DROP_ADAPTIVE_DT;
            } else {
                dropCount_ = 0;
                isRetain = true;
                lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_MAX_DROP;
            }
        } else {
            dropCount_ = 0;
            targetRatio_ = 1.0;
            isRetain = true;
            lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT;
        }
    } else {
        lastReason_ = SfdDecisionReason::UNSPECIFIED;
    }
    AVCODEC_LOGD("AdaptiveRetentionStrategy decision: %{public}d (speed:%{public}.3f, ratio:%{public}.3f)",
                 isRetain, currentSpeed_, targetRatio_);
    return isRetain;
}

void AdaptiveRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    currentSpeed_ = DoubleUtils::IsGreater(speed, 0.0) ? speed : 1.0;
}

void AdaptiveRetentionStrategy::Reset()
{
    dropCount_ = 0;
}

RetentionStrategyType AdaptiveRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::ADAPTIVE;
}

double AdaptiveRetentionStrategy::GetRatio() const
{
    return targetRatio_;
}

SfdDecisionReason AdaptiveRetentionStrategy::GetLastDecisionReason() const
{
    return lastReason_;
}

bool AdaptiveRetentionStrategy::RequiresMvAnalysis() const
{
    return true;
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
