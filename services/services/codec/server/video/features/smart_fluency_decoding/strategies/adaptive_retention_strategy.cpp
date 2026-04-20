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
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AdaptiveStrategy"};
constexpr double MAX_SPEED_FOR_RETENTION = 1.2;
constexpr double MAX_DROPS_MULTIPLIER_HIGH = 30.0;

struct DecisionNode {
    double threshold;
    int8_t featureIndex;
    int8_t left;
    int8_t right;
    int8_t classId;
    bool isLeaf;
};

constexpr int NUM_NODES = 15;

constexpr DecisionNode TREE_NODES[NUM_NODES] = {
    { 21.0108261108,  0,  1,  8, -1, false }, // node 0
    {  6.5326397419,  0,  2,  5, -1, false }, // node 1
    {  0.6923948228,  1,  3,  4, -1, false }, // node 2
    { -2.0000000000, -1, -1, -1,  0, true  }, // node 3
    { -2.0000000000, -1, -1, -1,  0, true  }, // node 4
    {  0.4727701843,  1,  6,  7, -1, false }, // node 5
    { -2.0000000000, -1, -1, -1,  1, true  }, // node 6
    { -2.0000000000, -1, -1, -1,  1, true  }, // node 7
    {  0.5533757210,  1,  9, 12, -1, false }, // node 8
    {  0.3526516557,  1, 10, 11, -1, false }, // node 9
    { -2.0000000000, -1, -1, -1,  1, true  }, // node 10
    { -2.0000000000, -1, -1, -1,  1, true  }, // node 11
    {  0.9898027778,  1, 13, 14, -1, false }, // node 12
    { -2.0000000000, -1, -1, -1,  1, true  }, // node 13
    { -2.0000000000, -1, -1, -1,  0, true  }, // node 14
};

class FluencyDecisionTree {
public:
    static bool Predict(double perceptualMagnitude, double motionConsistency)
    {
        double features[2] = {perceptualMagnitude, motionConsistency};
        int8_t nodeId = 0;
        while (!TREE_NODES[nodeId].isLeaf) {
            int8_t featIdx = TREE_NODES[nodeId].featureIndex;
            double thresh = TREE_NODES[nodeId].threshold;
            if (features[featIdx] <= thresh) {
                nodeId = TREE_NODES[nodeId].left;
            } else {
                nodeId = TREE_NODES[nodeId].right;
            }
        }
        return TREE_NODES[nodeId].classId != 0;
    }
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

AdaptiveRetentionStrategy::AdaptiveRetentionStrategy() noexcept
{
    AVCODEC_LOGD("AdaptiveRetentionStrategy created");
}

bool AdaptiveRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    bool isRetain = true;
    if (currentSpeed_ <= MAX_SPEED_FOR_RETENTION) {
        isRetain = true;
        lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_SPEED;
    } else if (ctx.mvStats && ctx.mvStats->validBlocks > 0) {
        double consistency = ctx.mvStats->motionConsistency;
        double magnitude = ctx.mvStats->perceptualMagnitude * currentSpeed_;
        bool retentionFlag = FluencyDecisionTree::Predict(magnitude, consistency);
        if (!retentionFlag) {
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
    AVCODEC_LOGD("AdaptiveRetentionStrategy decision: %{public}d (speed:%{public}.2f, ratio:%{public}.2f)",
                 isRetain, currentSpeed_, targetRatio_);
    return isRetain;
}

void AdaptiveRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    currentSpeed_ = speed > 0.0 ? speed : 1.0;
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
