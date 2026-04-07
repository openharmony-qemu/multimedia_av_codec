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

#ifndef I_FRAME_RETENTION_STRATEGY_H
#define I_FRAME_RETENTION_STRATEGY_H

#include <memory>
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

// ==========================================
// 1. 极其优雅的上下文结构体 (Parameter Object)
// ==========================================
struct FrameRetentionContext {
    int64_t ptsMs;            // 当前帧的媒体时间戳
    int64_t sysTimeMs;        // 当前系统的物理时间 (用于防突发)
    uint32_t frameCount;      // 累计帧序号
    int32_t preDroppedCount = 0; // 前置丢帧计数
    
    // 把之前 makeDecision 里的重度参数都包进来，按需取用
    const SFDMVStats* mvStats = nullptr;
};

// ==========================================
// 2. 丢帧器标准基类接口
// ==========================================
class IFrameRetentionStrategy {
public:
    virtual ~IFrameRetentionStrategy() = default;
    // 核心决策接口：入参永远只有一个 Context
    virtual bool MakeRetentionDecision(const FrameRetentionContext& ctx) = 0;

    // 动态倍速下发接口
    virtual void UpdatePlaybackSpeed(double speed) = 0;

    // 状态重置
    virtual void Reset() = 0;

    // 获取当前策略模式
    virtual FrameRetentionMode GetMode() const = 0;

    // 定期上报解码倍速和帧率供自适应算法参考
    virtual void OnDecodingPerformanceUpdate(double /*speed*/, double /*decFps*/) {};

    // 获取当前策略的保留率 (0.0~1.0)
    virtual double GetCurrentRetentionRatio() const = 0;
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif