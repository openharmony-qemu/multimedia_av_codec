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

#include "full_retention_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

bool FullRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    // 透传模式：不丢弃任何帧（包括非参考帧）
    return true;
}

void FullRetentionStrategy::Reset()
{
    // 无状态
}

void FullRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    // 不关心播放速度，因为永远返回 true
}

FrameRetentionMode FullRetentionStrategy::GetMode() const
{
    return FrameRetentionMode::FULL;
}

double FullRetentionStrategy::GetCurrentRetentionRatio() const
{
    return 1.0;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
