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
    (void)ctx;
    return true;
}

void FullRetentionStrategy::Reset()
{
}

void FullRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    (void)speed;
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
